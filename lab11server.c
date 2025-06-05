#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

enum
{
  MAX_CLIENTS = 32,
  MAX_NAME = 32,
  MAX_LINE = 512
};
enum
{
  PING_INT = 15,
  PONG_TMO = 50 // timeout
};

typedef struct
{
  int fd;
  char name[MAX_NAME];
  time_t last_seen; // czas ostatniego PONG
} Client;

static Client clients[MAX_CLIENTS]; // tablica klientów
static int listen_fd;

// helperki
static void die(const char *msg)
{
  perror(msg);
  exit(1);
}

// parser tekstu wysyłanego
static void say(int fd, const char *fmt, ...)
{
  char buf[MAX_LINE];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  if (n > 0)
    write(fd, buf, n);
}

static const char *nowstr(void)
{
  static char buf[32];
  time_t t = time(NULL);
  strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", localtime(&t));
  return buf;
}

static int add_client(int fd, const char *name)
{
  for (int i = 0; i < MAX_CLIENTS; ++i)
    if (clients[i].fd == -1) // wolny slot
    {
      clients[i].fd = fd;
      strncpy(clients[i].name, name, MAX_NAME - 1);
      clients[i].last_seen = time(NULL);
      return 0;
    }
  return -1;
}

static void drop_client(int i)
{
  close(clients[i].fd);
  clients[i].fd = -1;
}

static void broadcast(const char *sender, const char *txt, int except_fd)
{
  for (int i = 0; i < MAX_CLIENTS; ++i)
    if (clients[i].fd != -1 && clients[i].fd != except_fd)
      say(clients[i].fd, "MSG %s %s %s\n", nowstr(), sender, txt);
}

// main
int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "użycie: %s <port>\n", argv[0]);
    exit(1);
  }
  signal(SIGPIPE, SIG_IGN);
  for (int i = 0; i < MAX_CLIENTS; ++i)
    clients[i].fd = -1;

  // gniazdo słuchające TCP
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  int yes = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes); // reuseaddr ważne

  struct sockaddr_in sin = {.sin_family = AF_INET,
                            .sin_addr.s_addr = INADDR_ANY,
                            .sin_port = htons(atoi(argv[1]))};
  if (bind(listen_fd, (void *)&sin, sizeof sin) || listen(listen_fd, 8))
    die("bind/listen");

  fprintf(stderr, "[server] listening on port %s …\n", argv[1]);

  while (1)
  {
    fd_set rd;
    FD_ZERO(&rd);
    FD_SET(listen_fd, &rd);
    int maxfd = listen_fd;
    for (int i = 0; i < MAX_CLIENTS; ++i)
      if (clients[i].fd != -1)
      {
        FD_SET(clients[i].fd, &rd);
        if (clients[i].fd > maxfd)
          maxfd = clients[i].fd;
      }

    struct timeval tv = {.tv_sec = 1, .tv_usec = 0}; // operacje co sekunde
    select(maxfd + 1, &rd, NULL, NULL, &tv);

    // nowe połączenia
    if (FD_ISSET(listen_fd, &rd))
    {
      int cfd = accept(listen_fd, NULL, NULL);
      char line[MAX_LINE] = {0};
      int n = read(cfd, line, sizeof line - 1);
      if (n <= 0 || strncmp(line, "HELLO ", 6))
      {
        close(cfd);
        continue;
      }
      char *name = line + 6;
      name[strcspn(name, "\r\n")] = 0;
      if (add_client(cfd, name) == -1)
      {
        say(cfd, "ERR server full\n");
        close(cfd);
        continue;
      }
      fprintf(stderr, "[+] %s connected\n", name);
    }

    // przejście po klientach
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
      if (clients[i].fd == -1)
        continue;

      // timeout PONG, drop client
      if (now - clients[i].last_seen > PONG_TMO)
      {
        fprintf(stderr, "[-] %s timed-out\n", clients[i].name);
        drop_client(i);
        continue;
      }

      // czytanie z klienta
      if (FD_ISSET(clients[i].fd, &rd))
      {
        char line[MAX_LINE] = {0};
        int n = read(clients[i].fd, line, sizeof line - 1);
        if (n <= 0)
        {
          drop_client(i);
          continue;
        }

        // obsługa komendy
        line[strcspn(line, "\r\n")] = 0;
        if (!strcmp(line, "PONG"))
        {
          clients[i].last_seen = now;
        }
        else if (!strcmp(line, "LIST"))
        {
          say(clients[i].fd, "LIST ");
          for (int j = 0; j < MAX_CLIENTS; ++j)
            if (clients[j].fd != -1)
              say(clients[i].fd, "%s ", clients[j].name);
          say(clients[i].fd, "\n");
        }
        else if (!strncmp(line, "2ALL ", 5))
        {
          broadcast(clients[i].name, line + 5, -1);
        }
        else if (!strncmp(line, "2ONE ", 5))
        {
          char *who = strtok(line + 5, " ");
          char *txt = strtok(NULL, "");
          int found = 0;
          for (int j = 0; j < MAX_CLIENTS; ++j)
            if (clients[j].fd != -1 && !strcmp(clients[j].name, who))
            {
              say(clients[j].fd, "MSG %s %s %s\n",
                  nowstr(), clients[i].name, txt ? txt : "");
              found = 1;
              break;
            }
          if (!found)
            say(clients[i].fd, "ERR no-such-client\n");
        }
        else if (!strcmp(line, "STOP"))
        {
          drop_client(i);
        }
        else
        {
          say(clients[i].fd, "ERR unknown-cmd\n");
        }
      }
      // co PING_INT sekund wysyłamy PING
      if (now % PING_INT == 0 && now != clients[i].last_seen)
        say(clients[i].fd, "PING\n");
    }
  }
}
