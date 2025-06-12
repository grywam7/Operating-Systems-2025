#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Funkcja 'waitvar' przyjmuje:
 *   - zmienna warunkowa 'var',
 *   - zwiazany z nia mutex 'm',
 *   - wskaznik 'cond' na flage okreslajaca stan warunku.
 * Funkcja 'waitvar' powinna blokowac watek na zmiennej 'var'
 * tak dlugo jak flaga wskazywana przez 'cond' ma wartosc 0.
 * Wychodzac z funkcji 'waitvar' watek powinien miec zajety
 * mutex 'm'.
 */
void waitvar(pthread_mutex_t *m, pthread_cond_t *var, int *cond)
{
  if (pthread_mutex_lock(m) != 0)
    return;
  while (*cond == 0)
  {
    pthread_cond_wait(var, m);
  }
}

struct thr_args
{
  pthread_mutex_t m;
  pthread_cond_t var;
  int cond;
};

void *thr_entry(void *args)
{
  struct thr_args *a = (struct thr_args *)args;
  int lcond = 0;

  waitvar(&a->m, &a->var, &a->cond);
  printf("%d ", a->cond);
  if (pthread_mutex_unlock(&a->m))
    return NULL;
  while (!lcond)
  {
    if (pthread_mutex_lock(&a->m))
      return NULL;
    lcond = a->cond = rand() % 2;
    if (pthread_mutex_unlock(&a->m))
      return NULL;
    pthread_cond_signal(&a->var);
    sched_yield();
  }

  return NULL;
}

int main(int argc, char *argv[])
{
  struct thr_args a;
  pthread_mutexattr_t attr;
  pthread_t tid[20];
  int i;

  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&a.m, &attr);
  pthread_cond_init(&a.var, NULL);
  a.cond = 0;
  for (i = 0; i < 20; i++)
    pthread_create(tid + i, NULL, thr_entry, &a);
  sleep(1);
  a.cond = 1;
  pthread_cond_signal(&a.var);
  for (i = 0; i < 20; i++)
    pthread_join(tid[i], NULL);
  printf("%c\n", rand() % 26 + 'a');

  return 0;
}
