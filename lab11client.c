#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
  MAX_LINE = 512
};

static int sockfd = -1;
static atomic_int running = 1;

static void die(const char *msg)
{
  perror(msg);
  exit(1);
}

static void *socket_reader(void *arg)
{
  char buf[MAX_LINE];
  while (running)
  {
    int n = read(sockfd, buf, sizeof buf - 1);
    if (n <= 0)
    {
      running = 0;
      break;
    }
    buf[n] = 0;
    fputs(buf, stdout);
  }
  return NULL;
}

static void stop_and_exit(int sig)
{
  (void)sig;
  if (sockfd != -1)
    write(sockfd, "STOP\n", 5);
  running = 0;
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr, "użycie: %s <nazwa> <ip:port>\n", argv[0]);
    return 1;
  }
  const char *name = argv[1];
  char host[128];
  int port;
  sscanf(argv[2], "%127[^:]:%d", host, &port);

  // parsowanie adresu
  struct sockaddr_in sin = {.sin_family = AF_INET, .sin_port = htons(port)};
  if (inet_pton(AF_INET, host, &sin.sin_addr) != 1)
    die("inet_pton");

  // gniazdo TCP
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(sockfd, (void *)&sin, sizeof sin))
    die("connect");

  char hello[MAX_LINE];
  snprintf(hello, sizeof hello, "HELLO %s\n", name); // wysyłamy naszo nazwe
  write(sockfd, hello, strlen(hello));

  signal(SIGINT, stop_and_exit);

  pthread_t thr;
  pthread_create(&thr, NULL, socket_reader, NULL);

  // czytaj z stdin i wysyłaj
  char line[MAX_LINE];
  while (running && fgets(line, sizeof line, stdin))
  {
    if (!strncmp(line, "QUIT", 4))
      break;
    write(sockfd, line, strlen(line));
  }
  stop_and_exit(0);
  pthread_join(thr, NULL);
  close(sockfd);
}
