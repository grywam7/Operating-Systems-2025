#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MAX 20

int main(void)
{
  const char *shm_name = "/shm_zad2";
  int fd;
  int *buf;

  // Utwórz segment pamięci współdzielonej o prawach dostępu 600,
  // Jeśli segment już istnieje, to zwróć błąd i zakończ program z wartością 1
  // Jeśli utworzenie segmentu pamięci się nie powiedzie, to też zwróć błąd i zakończ program z wartością 1
  fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (fd == -1)
  {
    if (errno == EEXIST)
      fprintf(stderr, "Segment już istnieje\n");
    else
      perror("shm_open");
    return 1;
  }

  // rozmiarze MAX
  if (ftruncate(fd, MAX * sizeof *buf) == -1)
  {
    perror("ftruncate");
    shm_unlink(shm_name);
    return 1;
  }

  // Przyłącz segment pamięci współdzielonej do procesu, obsłuż błędy i zwróć 1 (w przypadku błędu)
  // Podłączając pamięć ustaw prawa dostępu do mapowanej pamięci: odczyt, zapis
  // Specygikacja użycia segmentu -  współdzielony przez procesy
  buf = mmap(NULL,
             MAX * sizeof *buf,
             PROT_READ | PROT_WRITE,
             MAP_SHARED,
             fd, 0);
  if (buf == MAP_FAILED)
  {
    perror("mmap");
    shm_unlink(shm_name);
    return 1;
  }

  for (int i = 0; i < MAX; i++)
  {
    buf[i] = i * i;
    printf("Wartość: %d\n", buf[i]);
  }
  printf("Memory written\n");

  // Odłącz i  usuń segment pamięci współdzielonej
  munmap(buf, MAX * sizeof *buf);
  close(fd);
  shm_unlink(shm_name);

  return 0;
}
