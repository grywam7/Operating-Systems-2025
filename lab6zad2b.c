#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>

static inline double f(double x) { return 4.0 / (x * x + 1); }

static double integrate(double a, double b, double dx)
{
  long long N = (long long)ceil((b - a) / dx);
  double sum = 0.0, x = a;
  for (long long i = 0; i < N; ++i, x += dx)
    sum += f(x) * dx;
  return sum;
}

int main(void)
{
  const char *in = "/tmp/fifo_req";
  const char *out = "/tmp/fifo_res";
  mkfifo(in, 0666);
  mkfifo(out, 0666);

  int fd_in = open(in, O_RDONLY);
  int fd_out = open(out, O_WRONLY);

  double a, b, dx;
  if (read(fd_in, &a, sizeof a) != sizeof a ||
      read(fd_in, &b, sizeof b) != sizeof b ||
      read(fd_in, &dx, sizeof dx) != sizeof dx)
  {
    fprintf(stderr, "worker: złe dane\n");
    exit(1);
  }
  fprintf(stderr, "worker: a=%g b=%g dx=%g\n", a, b, dx);

  double res = integrate(a, b, dx);
  write(fd_out, &res, sizeof res);

  close(fd_in);
  close(fd_out);
  return 0;
}
