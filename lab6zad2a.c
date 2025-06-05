#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    fprintf(stderr, "Użycie: %s <dx> <a> <b>\n"
                    "  <dx> - szerokość prostokąta (np. 1e-9)\n"
                    "  <a>  - początek przedziału całkowania\n"
                    "  <b>  - koniec przedziału całkowania\n",
            argv[0]);
    return 1;
  }
  double dx = strtod(argv[1], NULL),
         a = strtod(argv[2], NULL),
         b = strtod(argv[3], NULL);
  if (dx <= 0)
  {
    fputs("dx>0\n", stderr);
    return 1;
  }

  const char *in = "/tmp/fifo_req";
  const char *out = "/tmp/fifo_res";
  mkfifo(in, 0666);
  mkfifo(out, 0666);

  int fd_in = open(in, O_WRONLY);
  int fd_out = open(out, O_RDONLY);

  write(fd_in, &a, sizeof a);
  write(fd_in, &b, sizeof b);
  write(fd_in, &dx, sizeof dx);

  double res = 0.0;
  if (read(fd_out, &res, sizeof res) != sizeof res)
    fprintf(stderr, "master: błąd odczytu\n");
  else
    printf("∫[%g,%g] 4/(x²+1) dx ≈ %.15f\n", a, b, res);

  close(fd_in);
  close(fd_out);
  return 0;
}
