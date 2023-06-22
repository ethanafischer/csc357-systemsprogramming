#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_REQUEST_LENGTH 1062
#define MAX_KEY_LENGTH 32
#define MAX_VALUE_LENGTH 1024

int main(int argc, char *argv[]) {
	if (argc < 4) {
		printf("Usage: ./kvstore <namedPipe> <operation> <key> [<value>]\n");
		return 1;
	}

	const char *namedPipe = argv[1];
	const char *operation = argv[2];
	const char *key = argv[3];
	const char *value = (argc > 4) ? argv[4] : NULL;
	char clientFifo[16] = "";

    int fifo = open(namedPipe, O_WRONLY);

    char request[MAX_REQUEST_LENGTH];
    if (strcmp(operation, "set") == 0) {
        snprintf(request, sizeof(request), "%s %s %s\n", operation, key, value);
    } else if (strcmp(operation, "get") == 0) {
		pid_t pid = getpid();
		snprintf(clientFifo, sizeof(clientFifo), "%d", pid);
		snprintf(request, sizeof(request), "%s %d %s\n", operation, pid, key);
	} else {
		printf("Invalid command\n");
		return 1;
	}
	write(fifo, request, sizeof(request));
	close(fifo);

	if (strcmp(operation, "get") == 0) {
		//usleep(100);
		sleep(1);
		const char* NULL_STRING = "(null)";
		int clientFifoFD = open(clientFifo, O_RDONLY);
		
		char line[MAX_VALUE_LENGTH];
		ssize_t bytesRead = read(clientFifoFD, &line, sizeof(line)-1);
		line[bytesRead] = '\0';
        
		if (strcmp(line, NULL_STRING) == 0) {
			printf("Key %s does not exist.\n", key);
		} else {
			printf("%s\n", line);
		}
		close(clientFifoFD);
		unlink(clientFifo);
	}

	return 0;
}
