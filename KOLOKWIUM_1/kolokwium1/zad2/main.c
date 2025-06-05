#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void sighandler(siginfo_t *info, void *ucontext)
{
  (void)ucontext;
  printf("The value received from parent %d \n", info->si_value.sival_int);
}

int main(int argc, char *argv[])
{

  if (argc != 3)
  {
    printf("Not a suitable number of program parameters\n");
    return 1;
  }

  struct sigaction action;
  action.sa_sigaction = &sighandler;

  int value_to_send = atoi(argv[1]);
  int sig_to_send = atoi(argv[2]);

  action.sa_flags = SA_SIGINFO;
  sigemptyset(&action.sa_mask);

  int child = fork();
  if (child == 0)
  {

    // zablokuj wszystkie sygnaly za wyjatkiem SIGUSR1
    // zdefiniuj obsluge SIGUSR1 w taki sposob zeby proces potomny wydrukowal
    // na konsole przekazana przez rodzica wraz z sygnalem SIGUSR1 wartosc
    // CHILD PROCESS

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);

    if (sigprocmask(SIG_SETMASK, &mask, NULL) < 0)
    {
      perror("Child: sigprocmask");
      exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR1, &action, NULL) < 0)
    {
      perror("Child: sigaction");
      exit(EXIT_FAILURE);
    }

    while (1)
    {
      pause();
    }
  }
  else
  {
    // wyslij do procesu potomnego sygnal przekazany jako argv[2]
    // wraz z wartoscia przekazana jako argv[1]

    union sigval sigval_arg;
    sigval_arg.sival_int = value_to_send;
    // if (sigqueue(child, sig_to_send, sigval_arg) < 0)
    // {
    //   perror("Parent: sigqueue");
    //   exit(EXIT_FAILURE);
    // }
    if (kill(child, sig_to_send) < 0)
    {
      perror("Parent: kill");
      exit(EXIT_FAILURE);
    }
  }
  printf("Parent: Sent signal %d with value %d to child (PID %d).\n",
         sig_to_send, value_to_send, child);

  sleep(1);

  return 0;
}
