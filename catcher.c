// Niestety na MacOS sigqueue nie działa

#define _POSIX_C_SOURCE 200112L
#define _DARWIN_C_SOURCE
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

volatile sig_atomic_t current_mode = 0;
volatile sig_atomic_t new_signal = 0;
volatile sig_atomic_t count_mode1 = 0;
volatile sig_atomic_t sender_pid_global = 0;

void sigint_handler_custom(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "Wciśnięto CTRL+C\n", 18);
}

void catcher_handler(int signum, siginfo_t *info, void *context) {
    (void)context;
    int mode = 0;
    switch (signum) {
        case SIGUSR1: mode = 1; break;
        case SIGUSR2: mode = 2; break;
        case SIGQUIT: mode = 3; break;
        case SIGALRM:  mode = 4; break;
        case SIGTERM: mode = 5; break;
        default: mode = 0; break;
    }
    current_mode = mode;
    sender_pid_global = info->si_pid;
    new_signal = 1;

    if (kill(info->si_pid, SIGUSR1) == -1) {
        perror("Catcher: Błąd wysyłania potwierdzenia");
    }
}

int main(void) {
    printf("Catcher: PID = %d\n", getpid());

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = catcher_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) { perror("sigaction SIGUSR1"); exit(EXIT_FAILURE); }
    if (sigaction(SIGUSR2, &sa, NULL) == -1) { perror("sigaction SIGUSR2"); exit(EXIT_FAILURE); }
    if (sigaction(SIGQUIT, &sa, NULL) == -1) { perror("sigaction SIGQUIT"); exit(EXIT_FAILURE); }
    if (sigaction(SIGALRM, &sa, NULL) == -1) { perror("sigaction SIGALRM"); exit(EXIT_FAILURE); }
    if (sigaction(SIGTERM, &sa, NULL) == -1) { perror("sigaction SIGTERM"); exit(EXIT_FAILURE); }

    signal(SIGINT, SIG_DFL);

    while (1) {
        pause();
        if (new_signal) {
            new_signal = 0;
            switch (current_mode) {
                case 2:
                    printf("Mode 2: Rozpoczynam ciągłe wypisywanie liczb.\n");
                    int continuous_counter = 1;
                    while (current_mode == 2 && !new_signal) {
                        printf("%d\n", continuous_counter++);
                        sleep(1);
                    }
                case 1:
                    count_mode1++;
                    printf("Mode 1: Odebrano sygnał. Liczba odebranych sygnałów: %d\n", count_mode1);
                    break;
                case 3:
                    signal(SIGINT, SIG_IGN);
                    printf("Mode 3: Ignorowanie SIGINT zostało ustawione.\n");
                    break;
                case 4:
                    signal(SIGINT, sigint_handler_custom);
                    printf("Mode 4: Ustawiono handler dla SIGINT (wciśnięcie CTRL+C wyświetli komunikat).\n");
                    break;
                case 5:
                    printf("Mode 5: Zakończenie działania catchera.\n");
                    exit(0);
                    break;
                default:
                    break;
            }
        }
    }
    return 0;
}
