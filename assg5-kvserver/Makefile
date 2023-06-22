CC = gcc
CFLAGS = -Wall -Wextra

all: kvstore kvclient

program1: kvstore.c
	$(CC) $(CFLAGS) -o kvstore kvstore.c

program2: kvclient.c
	$(CC) $(CFLAGS) -o kvclient kvclient.c

clean:
	rm -f kvstore kvclient
