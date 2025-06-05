#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <math.h>

static inline double f(double x) { return 4.0 / (x * x + 1.0); }

static double timestamp(void)
{
  struct timeval timeValue;
  gettimeofday(&timeValue, NULL);
  return timeValue.tv_sec + timeValue.tv_usec * 1e-6;
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr,
            "Użycie: %s <dx> <n>\n"
            "  <dx> - szerokość prostokąta (np. 1e-9)\n"
            "  <n>  - maksymalna liczba procesów potomnych (k=1..n)\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  const double dx = strtod(argv[1], NULL);
  const int n = atoi(argv[2]);
  if (dx <= 0.0 || n <= 0)
  {
    fputs("Błędne argumenty.\n", stderr);
    return EXIT_FAILURE;
  }

  // prostokąty
  const long long N_total = (long long)ceil(1.0 / dx);

  for (int k = 1; k <= n; ++k)
  {

    // nowe potoki dla każdego procesu
    int pipes[k][2];
    for (int i = 0; i < k; ++i)
      if (pipe(pipes[i]) == -1)
      {
        perror("pipe");
        exit(EXIT_FAILURE);
      }

    const double t0 = timestamp();

    const long long rectsPerProcess = N_total / k;
    const long long extraRects = N_total % k;

    for (int i = 0; i < k; ++i)
    {
      long long local_N = rectsPerProcess + (i < extraRects);
      long long first = rectsPerProcess * i + (i < extraRects ? i : extraRects);
      pid_t pid = fork();
      if (pid == -1)
      {
        perror("fork");
        exit(EXIT_FAILURE);
      }

      if (pid == 0)
      {
        // dziecko
        close(pipes[i][0]);
        double sum = 0.0;
        double x = first * dx;
        for (long long j = 0; j < local_N; ++j, x += dx)
          sum += f(x) * dx;

        if (write(pipes[i][1], &sum, sizeof(sum)) != sizeof(sum))
          perror("write");
        close(pipes[i][1]);
        _exit(0);
      }

      // rodzic
      close(pipes[i][1]);
    }

    double integral = 0.0, part;
    for (int i = 0; i < k; ++i)
    {
      if (read(pipes[i][0], &part, sizeof(part)) == sizeof(part))
        integral += part;
      close(pipes[i][0]);
    }

    while (wait(NULL) > 0)
    {
    }

    const double dt = timestamp() - t0;

    printf("k=%d  wynik=%0.15f  czas=%0.3f s\n", k, integral, dt);
  }

  return 0;
}
