#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void downloadFile(char* outputFile, char* url, int maxSeconds, int lineNum) {
	char maxSecondsStr[10];
	snprintf(maxSecondsStr, sizeof(maxSecondsStr), "%d", maxSeconds);

	pid_t pid = fork();

	if (pid == 0) {
		execlp("curl", "curl", "-m", maxSecondsStr, "-o", outputFile, "-s", url, NULL);
		exit(1);
	} else {
		printf("Process %d processing line #%d\n", getpid(), lineNum);
		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			printf("Process %d finished line #%d\n", pid, lineNum);
		} else {
			printf("Error: Process %d terminated abnormally\n", getpid());
		}
	}
}

void downloadFiles(char* filename, int maxProcesses) {
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		return;
	}

	char line[256];
	int lineNum = 1;
	int activeProcesses = 0;

	while (fgets(line, sizeof(line), file)) {
		char* outputFile = strtok(line, " \n");
		char* url = strtok(NULL, " \n");
		int maxSeconds = 0;

		char* maxSecondsStr = strtok(NULL, " \n");
		if (maxSecondsStr != NULL) {
			maxSeconds = atoi(maxSecondsStr);
		}

		downloadFile(outputFile, url, maxSeconds, lineNum);
		activeProcesses++;

		if (activeProcesses >= maxProcesses) {
			wait(NULL);
			activeProcesses--;
		}

		lineNum++;
	}

	fclose(file);

	while (activeProcesses > 0) {
		wait(NULL);
		activeProcesses--;
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		return 1;
	}
	char* filename = argv[1];
	int maxProcesses = atoi(argv[2]);

	downloadFiles(filename, maxProcesses);
	
	return 0;
}
