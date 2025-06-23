#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MAX_CLIENTS 32
#define BUF 1024
#define PING_EVERY 5
#define PING_TIMEOUT 15

typedef struct
{
  char nick[32];
  struct sockaddr_in addr;
  time_t last_seen;
  int alive;
} client_t;

static client_t clients[MAX_CLIENTS];

static int find_by_nick(const char *n)
{
  for (int i = 0; i < MAX_CLIENTS; ++i)
    if (clients[i].nick[0] && strcmp(clients[i].nick, n) == 0)
      return i;
  return -1;
}

static int add_client(const char *nick, const struct sockaddr_in *a)
{
  if (find_by_nick(nick) != -1)
    return -1;
  for (int i = 0; i < MAX_CLIENTS; ++i)
  {
    if (clients[i].nick[0] == '\0')
    {
      strncpy(clients[i].nick, nick, sizeof(clients[i].nick) - 1);
      clients[i].addr = *a;
      clients[i].last_seen = time(NULL);
      clients[i].alive = 1;
      return i;
    }
  }
  return -1;
}

static void remove_client(int idx) { clients[idx].nick[0] = '\0'; }

static void broadcast(int sock, const char *msg, size_t len, int except)
{
  for (int i = 0; i < MAX_CLIENTS; ++i)
    if (clients[i].nick[0] && i != except)
      sendto(sock, msg, len, 0,
             (struct sockaddr *)&clients[i].addr, sizeof(clients[i].addr));
}

static void send_to(int sock, int idx, const char *msg, size_t len)
{
  sendto(sock, msg, len, 0,
         (struct sockaddr *)&clients[idx].addr, sizeof(clients[idx].addr));
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  int port = atoi(argv[1]);

  int s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0)
  {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in sin = {.sin_family = AF_INET,
                            .sin_addr.s_addr = INADDR_ANY,
                            .sin_port = htons(port)};
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    perror("bind");
    exit(1);
  }
  printf("Server on port %d\n", port);

  char buf[BUF];
  struct sockaddr_in cli;
  socklen_t clen = sizeof(cli);
  time_t last_ping = time(NULL);

  for (;;)
  {
    fd_set rd;
    FD_ZERO(&rd);
    FD_SET(s, &rd);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    select(s + 1, &rd, NULL, NULL, &tv);

    if (FD_ISSET(s, &rd))
    {
      int n = recvfrom(s, buf, sizeof(buf) - 1, 0,
                       (struct sockaddr *)&cli, &clen);
      if (n <= 0)
        continue;
      buf[n] = '\0';

      char cmd[16], arg1[64];
      if (sscanf(buf, "%15s %63s", cmd, arg1) < 1)
        continue;

      int idx = -1;
      // jeżeli nadawca jest znany → znajdź indeks
      for (int i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i].nick[0] &&
            memcmp(&clients[i].addr, &cli, sizeof(cli)) == 0)
        {
          idx = i;
          break;
        }

      // obsługa poleceń
      if (strcmp(cmd, "INIT") == 0)
      {
        if (add_client(arg1, &cli) >= 0)
          printf("++ %s joined\n", arg1);
      }
      else if (strcmp(cmd, "LIST") == 0 && idx != -1)
      {
        char out[BUF];
        int off = sprintf(out, "LIST");
        for (int i = 0; i < MAX_CLIENTS; ++i)
          if (clients[i].nick[0])
            off +=
                sprintf(out + off, " %s", clients[i].nick);
        out[off++] = '\n';
        out[off] = 0;
        send_to(s, idx, out, off);
      }
      else if (strcmp(cmd, "2ALL") == 0 && idx != -1)
      {
        char *msg = strchr(buf, ' ');
        if (msg)
          msg = strchr(msg + 1, ' ');
        if (msg)
        {
          char out[BUF];
          int off = snprintf(out, BUF, "%s: ", clients[idx].nick);
          strncpy(out + off, msg + 1, BUF - off - 1);
          broadcast(s, out, strlen(out), idx);
        }
      }
      else if (strcmp(cmd, "2ONE") == 0 && idx != -1)
      {
        char *msg = strchr(buf, ' ');
        char *msg2 = msg ? strchr(msg + 1, ' ') : NULL;
        if (msg && msg2)
        {
          *msg2 = 0;
          int to = find_by_nick(msg + 1);
          if (to != -1)
          {
            char out[BUF];
            int off = snprintf(out, BUF, "HEY %s ", clients[idx].nick);
            strncpy(out + off, msg2 + 1, BUF - off - 1);
            send_to(s, to, out, strlen(out));
          }
        }
      }
      else if (strcmp(cmd, "STOP") == 0 && idx != -1)
      {
        printf("-- %s left\n", clients[idx].nick);
        remove_client(idx);
      }
      else if (strcmp(cmd, "PONG") == 0 && idx != -1)
      {
        clients[idx].alive = 1;
        clients[idx].last_seen = time(NULL);
      }
    }

    // cykliczne PING oraz czyszczenie
    time_t now = time(NULL);
    if (now - last_ping >= PING_EVERY)
    {
      last_ping = now;
      for (int i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i].nick[0])
        {
          if (!clients[i].alive && now - clients[i].last_seen > PING_TIMEOUT)
          {
            printf("!! timeout %s\n", clients[i].nick);
            remove_client(i);
          }
          else
          {
            clients[i].alive = 0;
            send_to(s, i, "PING\n", 5);
          }
        }
    }
  }
}
