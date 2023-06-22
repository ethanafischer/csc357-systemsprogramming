#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

#define MAX_KEY_LENGTH 32
#define MAX_VALUE_LENGTH 1024
#define HASH_TABLE_SIZE 1000

typedef struct HashNode {
	char key[MAX_KEY_LENGTH];
	char value[MAX_VALUE_LENGTH];
	struct HashNode* next;
} HashNode;

HashNode* hashTable[HASH_TABLE_SIZE] = { NULL };
FILE* database;
char* fifoFile;
int fifo;


int hashFunction(const char* key) {
	int hash = 0;
	int len = strlen(key);
	for (int i = 0; i < len; i++) {
		hash = (hash * 31 + key[i]) % HASH_TABLE_SIZE;
	}
	return hash;
}


void insert(const char *key, const char *value) {
	int hashIndex = hashFunction(key);
	HashNode *currentNode = hashTable[hashIndex];

	while(currentNode != NULL) {
		if (strcmp(currentNode->key, key) == 0) {
			// key already exists, replace the value
			strcpy(currentNode->value, value);
			return;
		}
		// move to the next index
		currentNode = currentNode->next;
	}

	HashNode *item = (HashNode*) malloc(sizeof(HashNode));
	strcpy(item->key, key);
	strcpy(item->value, value);
	item->next = NULL;

	// key does not exist, create a new node and insert it
	if (hashTable[hashIndex] == NULL) {
		hashTable[hashIndex] = item;
	} else {
		item->next = hashTable[hashIndex];
		hashTable[hashIndex] = item;
	}
}


char* get(const char* key) {
	int index = hashFunction(key);
	HashNode* currentNode = hashTable[index];
    
	while (currentNode != NULL) {
		if (strcmp(currentNode->key, key) == 0) {
			// found key, return value
			return currentNode->value;
		}
		// go to next entry in hash table
		currentNode = currentNode->next;
	}

	// the key does not exist in the hash table
	return NULL;
}


void rebuildIndex(FILE* database) {
	// free hash table
	for (int i = 0; i < HASH_TABLE_SIZE; i++) {
		free(hashTable[i]);
		hashTable[i] = NULL;
    }

	char line[MAX_KEY_LENGTH + MAX_VALUE_LENGTH + 6];
	while (fgets(line, sizeof(line), database)) {
		// process current entries in database
		char key[MAX_KEY_LENGTH], value[MAX_VALUE_LENGTH];
		if (sscanf(line, "%[^,],%[^\n]", key, value) == 2) {
			insert(key, value);
        }
    }
}


void updateDatabase(const char* key, const char* value, FILE* database) {
	fseek(database, 0, SEEK_END);
	fprintf(database, "%s,%s\n", key, value);
	fflush(database);
}


void handleSIGQUIT(int sig) {
	signal(sig, SIG_IGN);
	fclose(database);
	close(fifo);
	unlink(fifoFile);
	exit(0);
}


void parseRequest(char* token, char* operation, char* key, char* value, char* pid) {
	int count = 1;
	while (token != NULL) {
		if (count == 1) {
			strcpy(operation, token);
		} else if (count == 2) {
			if (strncmp(operation, "set", 3) == 0) {
				strcpy(key, token);
			} else if (strncmp(operation, "get", 3) == 0) {
				strcpy(pid, token);
			}
		} else {
			if (strncmp(operation, "set", 3) == 0) {
				if (token[0] == '"') {
					strcpy(value, token + 1);
				} else {
					if (strlen(value) > 0) {
						strcat(value, " ");
					}
					strcat(value, token);
				}
			} else if (strncmp(operation, "get", 3) == 0) {
				if (token[0] == '"') {
					strcpy(key, token + 1);
				} else {
					strcat(key, token);
				}
			}
		}
		token = strtok(NULL, " ");
		count++;
	}

	if (strncmp(operation, "set", 3) == 0 && strlen(value) > 0) {
		value[strlen(value) - 1] = '\0';
	} else if (strncmp(operation, "get", 3) == 0) {
		key[strlen(key) - 1] = '\0';
	}
}


void printHashTable() {
	printf("\nHash Table Contents:\n");
	for (int i = 0; i < HASH_TABLE_SIZE; i++) {
		HashNode *currentNode = hashTable[i];
		if (currentNode != NULL) {
			printf("Key: %s, Value: %s\n", currentNode->key, currentNode->value);
		}
	}
}


int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("usage: ./kvstore <fifo name>");
		return 1;
    }
	
	char* databaseFile = argv[1];
	fifoFile = argv[2];

	// Open database file for read/write
	database = fopen(databaseFile, "a+");

	// Rebuild the index from the existing entries in the database file
	rebuildIndex(database);

	// Create/open FIFO
	mkfifo(fifoFile, 0666);
	fifo = open(fifoFile, O_RDONLY);

	// Register signal handler for SIGQUIT
	signal(SIGQUIT, handleSIGQUIT);

	//printHashTable();

	char request[MAX_KEY_LENGTH + MAX_VALUE_LENGTH + 6];
	ssize_t bytesRead;

	while (1) {
		// Read FIFO request
		bytesRead = read(fifo, request, sizeof(request));

		if (bytesRead > 0) {
			request[bytesRead] = '\0';

			// Parse the request
			char* token = strtok(request, " ");
			char operation[4] = "";
			char key[MAX_KEY_LENGTH] = "";
			char value[MAX_VALUE_LENGTH] = "";
			char pid[16] = "";

			parseRequest(token, operation, key, value, pid);
			
			if (strcmp(operation, "get") == 0) {
				char* getValue = get(key);
				const char* NULL_VALUE = "(null)";
				
				// make client specific fifo (i.e. pid)
				mkfifo(pid, 0666);
				int clientFifoFD = open(pid, O_WRONLY);

				if (getValue == NULL) {
					write(clientFifoFD, NULL_VALUE, strlen(NULL_VALUE));
				} else {
					getValue[strlen(getValue)] = '\0';
					write(clientFifoFD, getValue, strlen(getValue)+1);
				}

				close(clientFifoFD);
			} 

			else if (strcmp(operation, "set") == 0) {
				insert(key, value);
				updateDatabase(key, value, database);
			}
		}
	}
	
	return 0;
}
