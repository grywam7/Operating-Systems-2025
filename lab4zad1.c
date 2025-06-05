#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Użycie: %s <liczba_procesow>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Podano nieprawidłową liczbę procesów (%d)\n", n);
        return 1;
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        } else if (pid == 0) {
            printf("Rodzic PID: %d, Potomek PID: %d\n", getppid(), getpid());
            // kończymy proces, by nie tworzył kolejnych
            _exit(0);
        }
        // Proces macierzysty przechodzi do kolejnej iteracji pętli
    }

    // Proces macierzysty czeka na zakończenie wszystkich potomków
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    // wypisujemy n
    printf("%d\n", n);

    return 0;
}
