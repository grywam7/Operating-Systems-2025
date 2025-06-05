#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

void sigusr1_handler(int signum) {
    printf("Odebrano sygnał SIGUSR1 (obsługa przez handler)!\n");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s {none|ignore|handler|mask}\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "none") == 0) {
        printf("Tryb: none. Użycie domyślnej obsługi SIGUSR1.\n");
    } else if (strcmp(argv[1], "ignore") == 0) {
        printf("Tryb: ignore. Ignorowanie sygnału SIGUSR1.\n");
        signal(SIGUSR1, SIG_IGN);
    } else if (strcmp(argv[1], "handler") == 0) {
        printf("Tryb: handler. Instalowanie funkcji obsługi SIGUSR1.\n");
        signal(SIGUSR1, sigusr1_handler);
    } else if (strcmp(argv[1], "mask") == 0) {
        printf("Tryb: mask. Maskowanie (blokowanie) sygnału SIGUSR1.\n");
        sigset_t new_mask, old_mask;
        sigemptyset(&new_mask);
        sigaddset(&new_mask, SIGUSR1);
        if (sigprocmask(SIG_BLOCK, &new_mask, &old_mask) < 0) {
            perror("Błąd sigprocmask");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Nieprawidłowy tryb: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    printf("Wysyłam sygnał SIGUSR1 do samego siebie.\n");
    raise(SIGUSR1);

    if (strcmp(argv[1], "mask") == 0) {
        sigset_t pending;
        if (sigpending(&pending) < 0) {
            perror("Błąd sigpending");
            exit(EXIT_FAILURE);
        }
        if (sigismember(&pending, SIGUSR1)) {
            printf("Sygnał SIGUSR1 jest w kolejce (pending).\n");
        } else {
            printf("Sygnał SIGUSR1 nie jest w kolejce.\n");
        }
    }

    printf("Program zakończył działanie.\n");
    return 0;
}
