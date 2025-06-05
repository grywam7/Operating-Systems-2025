#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

// odwraca kolejność znaków w stringu
// jeśli linia kończy się znakiem nowej linii, zostawiamy go na jej końcu
void reverse_string(char *str) {
    int len = strlen(str);
    int has_newline = 0;
    if (len > 0 && str[len - 1] == '\n') {
        has_newline = 1;
        str[len - 1] = '\0';
        len--;
    }
    for (int i = 0, j = len - 1; i < j; i++, j--) {
        char tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
    }
    if (has_newline) {
        str[len] = '\n';
        str[len + 1] = '\0';
    }
}

int is_text_file(const char *filename) {
    const char *dot = strrchr(filename, '.');
    return (dot && strcmp(dot, ".txt") == 0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Użycie: %s <katalog_źródłowy> <katalog_wynikowy>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_dir = argv[1];
    const char *dest_dir = argv[2];

    // zrobienie katalogu wynikowego, jeśli nie istnieje.
    if (mkdir(dest_dir, 0755) == -1 && errno != EEXIST) {
        perror("mkdir");
        return EXIT_FAILURE;
    }

    DIR *dp = opendir(src_dir);
    if (dp == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    char src_path[1024], dest_path[1024];

    // Przetwarzamy pliki z katalogu źródłowego
    while ((entry = readdir(dp)) != NULL) {
        // Przetwarzamy tylko pliki z rozszerzeniem ".txt"
        if (!is_text_file(entry->d_name))
            continue;

        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        FILE *fin = fopen(src_path, "r");
        if (!fin) {
            perror("fopen (plik źródłowy)");
            continue;
        }
        FILE *fout = fopen(dest_path, "w");
        if (!fout) {
            perror("fopen (plik wynikowy)");
            fclose(fin);
            continue;
        }

        char buffer[4096];
        // Odczyt linii z pliku, odwrócenie ich oraz zapis do pliku wynikowego
        while (fgets(buffer, sizeof(buffer), fin)) {
            reverse_string(buffer);
            fputs(buffer, fout);
        }

        fclose(fin);
        fclose(fout);
    }

    closedir(dp);
    return EXIT_SUCCESS;
}
