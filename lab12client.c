#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF 1024
static int sock;
static struct sockaddr_in serv;

static void bye(int sig)
{
  (void)sig;
  sendto(sock, "STOP\n", 5, 0,
         (struct sockaddr *)&serv, sizeof(serv));
  close(sock);
  exit(0);
}

static void send_cmd(const char *fmt, ...)
{
  char buf[BUF];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUF, fmt, ap);
  va_end(ap);
  size_t len = strlen(buf);
  if (buf[len - 1] != '\n')
  {
    buf[len] = '\n';
    buf[len + 1] = 0;
  }
  sendto(sock, buf, strlen(buf), 0,
         (struct sockaddr *)&serv, sizeof(serv));
}

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    fprintf(stderr, "usage: %s <nick> <host> <port>\n", argv[0]);
    exit(1);
  }
  const char *nick = argv[1];
  const char *host = argv[2];
  int port = atoi(argv[3]);

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  memset(&serv, 0, sizeof(serv));
  serv.sin_family = AF_INET;
  serv.sin_port = htons(port);
  if (inet_pton(AF_INET, host, &serv.sin_addr) != 1)
  {
    fprintf(stderr, "bad host\n");
    exit(1);
  }

  char init[64];
  snprintf(init, sizeof(init), "INIT %s\n", nick);
  sendto(sock, init, strlen(init), 0,
         (struct sockaddr *)&serv, sizeof(serv));

  signal(SIGINT, bye);

  printf("== chat started, commands: LIST / 2ALL txt / 2ONE nick txt ==\n");

  for (;;)
  {
    fd_set rd;
    FD_ZERO(&rd);
    FD_SET(sock, &rd);
    FD_SET(STDIN_FILENO, &rd);
    select(sock + 1, &rd, NULL, NULL, NULL);

    if (FD_ISSET(STDIN_FILENO, &rd))
    {
      char line[BUF];
      if (!fgets(line, sizeof(line), stdin))
        bye(0);
      if (strncmp(line, "LIST", 4) == 0)
        send_cmd("LIST");
      else if (strncmp(line, "2ALL ", 5) == 0)
        send_cmd("%s", line);
      else if (strncmp(line, "2ONE ", 5) == 0)
        send_cmd("%s", line);
      else
        fprintf(stderr, "bad cmd\n");
    }
    if (FD_ISSET(sock, &rd))
    {
      char buf[BUF];
      int n = recv(sock, buf, sizeof(buf) - 1, 0);
      if (n <= 0)
        continue;
      buf[n] = 0;
      if (strcmp(buf, "PING\n") == 0)
        send_cmd("PONG");
      else
        fputs(buf, stdout);
    }
  }
}
