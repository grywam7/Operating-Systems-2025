#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

volatile sig_atomic_t confirmation_received = 0;

void sender_confirmation_handler(int sig, siginfo_t *info, void *context) {
    (void)sig; (void)info; (void)context;
    confirmation_received = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <PID catchera> <mode (1-5)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t catcher_pid = (pid_t)atoi(argv[1]);
    int mode = atoi(argv[2]);

    int sig_to_send = 0;
    switch (mode) {
        case 1: sig_to_send = SIGUSR1; break;
        case 2: sig_to_send = SIGUSR2; break;
        case 3: sig_to_send = SIGQUIT; break;
        case 4: sig_to_send = SIGALRM; break;
        case 5: sig_to_send = SIGTERM; break;
        default:
            fprintf(stderr, "Nieprawidłowy tryb: %d. Dozwolone wartości to 1-5.\n", mode);
            exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sender_confirmation_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Sender: sigaction");
        exit(EXIT_FAILURE);
    }

    if (kill(catcher_pid, sig_to_send) == -1) {
        perror("Sender: kill");
        exit(EXIT_FAILURE);
    }
    printf("Sender: Wysłano sygnał (tryb %d) do catchera (PID %d). Oczekiwanie na potwierdzenie...\n", mode, catcher_pid);

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    if (sigprocmask(SIG_BLOCK, &mask, &oldmask) == -1) {
        perror("Sender: sigprocmask");
        exit(EXIT_FAILURE);
    }
    while (!confirmation_received) {
        sigsuspend(&oldmask);
    }
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) == -1) {
        perror("Sender: przywracanie maski sygnałów");
        exit(EXIT_FAILURE);
    }

    printf("Sender: Potwierdzenie odebrane. Kończę działanie.\n");
    return 0;
}
