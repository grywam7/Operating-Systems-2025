# Kompilator i flagi kompilacji
CC = gcc
CFLAGS = -Wall -std=c17

# collatz
COLLATZ_SOURCE = collatz.c
COLLATZ_HEADER = collatz.h

# biblioteki
STATIC_LIB = libcollatz.a
SHARED_LIB = libcollatz.so

# targety
all: countdown static shared client_static client_shared client_dynamic flipper lab4zad1 lab4zad2 lab5zad1 sender catcher lab6zad1 lab6zad2a lab6zad2b lab7client lab7server lab8client lab8server lab8printer lab9zad1 lab10zad1 lab11client lab11server lab12server lab12client

countdown: countdown.c
	$(CC) $(CFLAGS) -g -o countdown countdown.c

# biblioteka statyczna
static: $(COLLATZ_SOURCE) $(COLLATZ_HEADER)
	$(CC) $(CFLAGS) -c $(COLLATZ_SOURCE)
	ar rcs $(STATIC_LIB) collatz.o

# biblioteka wspoldzielona
shared: $(COLLATZ_SOURCE) $(COLLATZ_HEADER)
	$(CC) $(CFLAGS) -fPIC -c $(COLLATZ_SOURCE)
	$(CC) -shared -o $(SHARED_LIB) collatz.o

# klient z dynamiczna bibliota
client_dynamic: $(COLLATZ_SOURCE)
	$(CC) $(CFLAGS) -o client_dynamic client.c -ldl -DLOAD_DYNAMICALLY

# klient z biblioteka statyczna
client_static: client.c $(STATIC_LIB)
	$(CC) $(CFLAGS) -o client_static client.c -L. -lcollatz

# klient z biblioteka współdzielona
client_shared: client.c $(SHARED_LIB)
	$(CC) $(CFLAGS) -o client_shared client.c -L. -lcollatz -Wl,-rpath,.

flipper: flipper.c
	$(CC) $(CFLAGS) -o flipper flipper.c

lab4zad1: lab4zad1.c
	$(CC) $(CFLAGS) -o lab4zad1 lab4zad1.c

lab4zad2: lab4zad2.c
	$(CC) $(CFLAGS) -o lab4zad2 lab4zad2.c

lab5zad1: lab5zad1.c
	$(CC) $(CFLAGS) -o lab5zad1 lab5zad1.c

sender: sender.c
	$(CC) $(CFLAGS) -o sender sender.c

catcher: catcher.c
	$(CC) $(CFLAGS) -o catcher catcher.c

lab6zad1: lab6zad1.c
	$(CC) $(CFLAGS) -o lab6zad1 lab6zad1.c

lab6zad2a: lab6zad2a.c
	$(CC) $(CFLAGS) -o lab6zad2a lab6zad2a.c

lab6zad2b: lab6zad2b.c
	$(CC) $(CFLAGS) -o lab6zad2b lab6zad2b.c

lab7client: lab7client.c
	$(CC) $(CFLAGS) -o lab7client lab7client.c

lab7server.c: lab7server.c
	$(CC) $(CFLAGS) -o lab7server lab7server.c

lab8client: lab8client.c
	$(CC) $(CFLAGS) -o lab8client lab8client.c

lab8server: lab8server.c
	$(CC) $(CFLAGS) -o lab8server lab8server.c

lab8printer: lab8printer.c
	$(CC) $(CFLAGS) -o lab8printer lab8printer.c

lab9zad1: lab9zad1.c
	$(CC) $(CFLAGS) -o lab9zad1 lab9zad1.c

lab10zad1: lab10zad1.c
	$(CC) $(CFLAGS) -o lab10zad1 lab10zad1.c

lab11server: lab11server.c
	$(CC) $(CFLAGS) -o lab11server lab11server.c

lab11client: lab11client.c
	$(CC) $(CFLAGS) -o lab11client lab11client.c

lab12server: lab12server.c
	$(CC) $(CFLAGS) -o lab12server lab12server.c

lab12client: lab12client.c
	$(CC) $(CFLAGS) -o lab12client lab12client.c

clean:
	rm -f client_static client_shared client_dynamic countdown flipper $(STATIC_LIB) $(SHARED_LIB) collatz.o lab4zad1 lab4zad2 lab5zad1 sender catcher /tmp/fifo_req /tmp /fifo_res lab6zad1 lab6zad2a lab6zad2b  lab7client lab7server lab8client lab8server lab8printer lab9zad1 lab10zad1 lab11server lab11client lab12server lab12client

.PHONY: all clean static shared client_static client_shared client_dynamic 
