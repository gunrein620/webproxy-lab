CC = gcc
CFLAGS = -g -Wall -Wextra -std=gnu11
LDLIBS = -lpthread

all: echo_serv echo_cli

echo_serv: echo_serv.c echo.c csapp.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

echo_cli: echo_cli.c csapp.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf echo_serv echo_cli *.dSYM
