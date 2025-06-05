#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void msleep(int ms) { usleep(ms * 1000); }

static void timestamp(char *buf, size_t n)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm tm;
  localtime_r(&ts.tv_sec, &tm);
  strftime(buf, n, "%H:%M:%S", &tm);
}

static void logf(const char *fmt, ...)
{
  char tbuf[16];
  timestamp(tbuf, sizeof tbuf);
  printf("[%s] ", tbuf);
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  putchar('\n');
}

// zasoby współdzielone
#define ROOM_CAP 3

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_doctor = PTHREAD_COND_INITIALIZER; // budzenie lekarza
static pthread_cond_t cond_done = PTHREAD_COND_INITIALIZER;   // koniec wizyty

static int waiting_cnt = 0;
static int waiting_ids[ROOM_CAP];
static int patients_left = 0;
static int all_finished = 0; // flaga

// wątek pacjenta
static void *patient_thread(void *arg)
{
  int id = (intptr_t)arg;
  srand((unsigned)time(NULL) ^ (id << 8));

  // losowy czas 2-5 s
  msleep(2000 + rand() % 3001);
  logf("Pacjent(%d): Ide do szpitala", id);

  while (1)
  {
    pthread_mutex_lock(&mtx);

    if (waiting_cnt == ROOM_CAP) // poczekalnia pełna
    {
      int d = 2000 + rand() % 2001;
      logf("Pacjent(%d): za dużo pacjentów, wracam później za %d ms", id, d);
      pthread_mutex_unlock(&mtx);
      msleep(d);
      continue;
    }

    // siadam w poczekalni
    waiting_ids[waiting_cnt++] = id;
    logf("Pacjent(%d): czeka %d pacjentów na lekarza", id, waiting_cnt);

    if (waiting_cnt == ROOM_CAP)
    {
      logf("Pacjent(%d): budzę lekarza", id);
      pthread_cond_signal(&cond_doctor);
    }

    // czekam na sygnał zakończenia wizyty
    while (waiting_cnt != 0)
      pthread_cond_wait(&cond_done, &mtx);

    pthread_mutex_unlock(&mtx);
    logf("Pacjent(%d): kończę wizytę", id);
    break;
  }

  // aktualizacja licznika pozostałych pacjentów
  pthread_mutex_lock(&mtx);
  if (--patients_left == 0)
    pthread_cond_signal(&cond_doctor);
  pthread_mutex_unlock(&mtx);
  return NULL;
}

// wątek lekarza
static void *doctor_thread(void *arg)
{
  (void)arg;
  while (1)
  {
    pthread_mutex_lock(&mtx);

    while (waiting_cnt < ROOM_CAP && patients_left > 0)
      pthread_cond_wait(&cond_doctor, &mtx);

    if (waiting_cnt < ROOM_CAP && patients_left == 0)
    {
      all_finished = 1;
      pthread_mutex_unlock(&mtx);
      break; // koniec pracy lekarza
    }

    int ids[ROOM_CAP];
    memcpy(ids, waiting_ids, sizeof ids);

    logf("Lekarz: budzę się");
    logf("Lekarz: konsultuję pacjentów %d, %d, %d",
         ids[0], ids[1], ids[2]);
    pthread_mutex_unlock(&mtx);

    msleep(2000 + rand() % 2001); // 2-4 s konsultacji

    pthread_mutex_lock(&mtx);
    waiting_cnt = 0;                    // poczekalnia pusta
    pthread_cond_broadcast(&cond_done); // wypuść trójkę
    logf("Lekarz: zasypiam");
    pthread_mutex_unlock(&mtx);
  }
  logf("Lekarz: wszyscy pacjenci obsłużeni - kończę dyżur");
  return NULL;
}

// main
int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Użycie: %s <liczba_pacjentów>\n", argv[0]);
    return 1;
  }
  int P = atoi(argv[1]);
  if (P <= 0)
  {
    fputs("liczba pacjentów > 0!\n", stderr);
    return 1;
  }

  patients_left = P;
  srand((unsigned)time(NULL));

  pthread_t doc;
  pthread_t *pat = calloc(P, sizeof *pat);

  pthread_create(&doc, NULL, doctor_thread, NULL);

  for (int i = 0; i < P; ++i)
    pthread_create(&pat[i], NULL, patient_thread, (void *)(intptr_t)i);

  for (int i = 0; i < P; ++i)
    pthread_join(pat[i], NULL);

  pthread_join(doc, NULL);
  free(pat);
  return 0;
}
