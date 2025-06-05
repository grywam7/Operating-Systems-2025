#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

static inline double f(double x) { return 4.0 / (x * x + 1.0); }

struct Task
{
  long long first; // pierwszy prostokąt
  long long count; // ilość prostokątów
  double dx;       // szerokość prostokąta
  int id;          // numer w tablicy
};

// tablice globalne
static double *results = NULL;
static int *ready = NULL;

static void *worker(void *arg)
{
  struct Task *t = arg;
  double sum = 0.0, x = t->first * t->dx;
  for (long long i = 0; i < t->count; ++i, x += t->dx)
    sum += f(x) * t->dx;

  // zapis i zgłoszenie gotowości
  results[t->id] = sum;
  ready[t->id] = 1;
  return NULL;
}

static double now_sec(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr,
            "Użycie: %s <dx> <k>\n"
            "  <dx> - szerokość prostokąta, np. 1e-8\n"
            "  <k>  - liczba wątków\n",
            argv[0]);
    return 1;
  }
  double dx = strtod(argv[1], NULL);
  int k = atoi(argv[2]);
  if (dx <= 0 || k <= 0)
  {
    fputs("Błędne argumenty.\n", stderr);
    return 1;
  }

  // alokacja tablic wyników i gotowości
  results = calloc(k, sizeof(*results));
  ready = calloc(k, sizeof(*ready));

  pthread_t tid[k];
  struct Task tasks[k];

  // prostokąty
  long long N = (long long)ceil(1.0 / dx);

  long long base = N / k;
  long long remain = N % k;

  double t0 = now_sec();

  // tworzenie wątków
  for (int i = 0; i < k; ++i)
  {
    tasks[i].count = base + (i < remain);
    tasks[i].first = base * i + (i < remain ? i : remain);
    tasks[i].dx = dx;
    tasks[i].id = i;
    if (pthread_create(&tid[i], NULL, worker, &tasks[i]))
    {
      perror("pthread_create");
      return 2;
    }
  }

  // czekamy na gotowość wszystkich wątków
  int all;
  do
  {
    all = 1;
    for (int i = 0; i < k; ++i)
    {
      if (!ready[i])
      {
        all = 0;
        break;
      }
    }
  } while (!all);

  // wynik
  double integral = 0.0;
  for (int i = 0; i < k; ++i)
  {
    integral += results[i];
  }

  double dt = now_sec() - t0;
  printf("k=%d  wynik=%.15f  czas=%.2f s\n", k, integral, dt);

  free(results);
  free(ready);
  return 0;
}