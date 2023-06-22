#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define MAX_INODES 1024
#define MAX_NAME_LENGTH 32


typedef struct {
    uint32_t index;
    char type;
} Inode;


char *uint32_to_str(uint32_t i) {
    int length = snprintf(NULL, 0, "%lu", (unsigned long) i); // pretend to print to a string to determine length
    char *str = malloc(length + 1); // allocate space for the actual string
    snprintf(str, length + 1, "%lu", (unsigned long) i); // print to string

    return str;
}


void list_contents(int32_t currentDirectoryIndex) {
    // Open the current directory's inode file
    FILE *currentDirInodeFile = fopen(uint32_to_str(currentDirectoryIndex), "rb");
    if (!currentDirInodeFile) {
        fprintf(stderr, "Error: Failed to open current directory's inode file.\n");
        return;
    }

	// Read the contents of the current directory's inode file
    uint32_t inodeIndex;
    char dirName[MAX_NAME_LENGTH];
    while (fread(&inodeIndex, sizeof(uint32_t), 1, currentDirInodeFile) == 1 &&
           fread(dirName, MAX_NAME_LENGTH, 1, currentDirInodeFile) == 1) {
        printf("%u %s\n", inodeIndex, dirName);
    }

    fclose(currentDirInodeFile);
}


void make_directory(const char *name, Inode inodes[MAX_INODES], size_t *numInodes, uint32_t currentDirectoryIndex) {
    // Check if the given directory name already exists in the current directory
    for (size_t i = 0; i < *numInodes; i++) {
        if (inodes[i].type == 'd' && strcmp(name, uint32_to_str(inodes[i].index)) == 0) {
            fprintf(stderr, "Error: Directory '%s' already exists.\n", name);
            return;
        }
    }

    if (*numInodes >= MAX_INODES) {
        printf("Error: Maximum number of inodes reached.\n");
        return;
    }

    // Get the next available inode index
    uint32_t newInodeIndex = *numInodes;

    // Add the new directory entry to the current directory's inode file
    FILE *currentDirInodeFile = fopen(uint32_to_str(currentDirectoryIndex), "ab");
    fwrite(&newInodeIndex, sizeof(uint32_t), 1, currentDirInodeFile);
    fwrite(name, MAX_NAME_LENGTH, 1, currentDirInodeFile);
    fclose(currentDirInodeFile);

    // Update the inode list with the new directory entry
    inodes[*numInodes].index = newInodeIndex;
    inodes[*numInodes].type = 'd';
    (*numInodes)++;

	// Create the new directory's inode file with default entries
    FILE *newDirInodeFile = fopen(uint32_to_str(newInodeIndex), "wb");
    uint32_t selfIndex = newInodeIndex;
    uint32_t parentIndex = currentDirectoryIndex;
    fwrite(&selfIndex, sizeof(uint32_t), 1, newDirInodeFile);
    fwrite(".", MAX_NAME_LENGTH, 1, newDirInodeFile);
    fwrite(&parentIndex, sizeof(uint32_t), 1, newDirInodeFile);
    fwrite("..", MAX_NAME_LENGTH, 1, newDirInodeFile);
    fclose(newDirInodeFile);
}


void make_file(const char *name, Inode inodes[MAX_INODES], size_t *numInodes, uint32_t currentDirectoryIndex) {
    // Check if the file already exists in the current directory
    for (size_t i = 0; i < *numInodes; i++) {
        if (inodes[i].type == 'f' && strcmp(name, uint32_to_str(inodes[i].index)) == 0) {
            return;
        }
    }

    if (*numInodes >= MAX_INODES) {
        fprintf(stderr, "Error: Maximum number of inodes reached.\n");
        return;
    }

    // Get the next available inode index
    uint32_t newInodeIndex = *numInodes;

    // Add the new file entry to the current directory's inode file
    FILE *currentDirInodeFile = fopen(uint32_to_str(currentDirectoryIndex), "ab");
    fwrite(&newInodeIndex, sizeof(uint32_t), 1, currentDirInodeFile);
    fwrite(name, MAX_NAME_LENGTH, 1, currentDirInodeFile);
    fclose(currentDirInodeFile);

    // Update the inode list with the new file entry
    inodes[*numInodes].index = newInodeIndex;
    inodes[*numInodes].type = 'f';
    (*numInodes)++;

    // Create the new file (for simulation/debugging purposes)
    FILE *newFile = fopen(uint32_to_str(newInodeIndex), "w");
    fprintf(newFile, "%s\n", name);
	fclose(newFile);
}


void exit_program(Inode inodes[MAX_INODES], size_t numInodes) {
	FILE *inodesListFile = fopen("inodes_list", "wb");
    if (inodesListFile) {
        for (size_t i = 0; i < numInodes; i++) {
            fwrite(&(inodes[i].index), sizeof(uint32_t), 1, inodesListFile);
			fwrite(&(inodes[i].type), sizeof(char), 1, inodesListFile);
        }
        fclose(inodesListFile);
    }  
    // Exit the program
    exit(0);
}

void loadInodeMetadata(Inode inodes[MAX_INODES], size_t* numInodes) {
    FILE* inodesListFile = fopen("inodes_list", "rb");
    if (!inodesListFile) {
        fprintf(stderr, "Failed to open inodes_list file.\n");
        *numInodes = 0;
        return;
    }

    *numInodes = 0;
    int c;
    while ((c = fgetc(inodesListFile)) != EOF) {
        if (c == 'd' || c == 'f') {
            inodes[*numInodes].index = *numInodes;
            inodes[*numInodes].type = c;
            (*numInodes)++;
        }
    }

    fclose(inodesListFile);
}


void processInodeMetadata(const Inode inodes[MAX_INODES], size_t numInodes) {
    for (size_t i = 0; i < numInodes; ++i) {
        uint32_t number = inodes[i].index;
        char type = inodes[i].type;

        // Process the valid inode number and type
        printf("Inode Number: %u, Type: %c\n", number, type);
    }
}


void change_directory(const char *name, uint32_t *currentDirectoryIndex) {
	// Open the current directory's inode file
    FILE *currentDirInodeFile = fopen(uint32_to_str(*currentDirectoryIndex), "rb");
    if (!currentDirInodeFile) {
        fprintf(stderr, "Error: Failed to open current directory's inode file.\n");
        return;
    }

    // Read the contents of the current directory's inode file
    uint32_t inodeIndex;
    char dirName[MAX_NAME_LENGTH];
    while (fread(&inodeIndex, sizeof(uint32_t), 1, currentDirInodeFile) == 1 &&
           fread(dirName, MAX_NAME_LENGTH, 1, currentDirInodeFile) == 1) {
        if (strcmp(dirName, name) == 0) {
            // Found the directory with the given name
            fclose(currentDirInodeFile);
            *currentDirectoryIndex = inodeIndex;
            return;
        }
    }

    fclose(currentDirInodeFile);
    printf("Directory '%s' not found.\n", name);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file_system_directory>\n", argv[0]);
        exit(1);
    }

    const char *directoryPath = argv[1];
	chdir(directoryPath);
	
    Inode inodes[MAX_INODES];
    size_t numInodes = 0;

    loadInodeMetadata(inodes, &numInodes);
	
	//processInodeMetadata(inodes, numInodes);

	// initialize root directory
	uint32_t currentDirectoryIndex = 0;
	if (inodes[currentDirectoryIndex].type != 'd') {
		fprintf(stderr, "Error: Invalid root directory.\n");
		return 1;
	}
		
	char line[1024];

    while (1) {
        printf("> ");
        fgets(line, sizeof(line), stdin);
		line[strcspn(line, "\n")] = '\0';  // Remove trailing newline character
		
		// parse the line
		char cmd[1024] = "";
		char arg[1024] = "";

		char *token = strtok(line, " ");
		if (token != NULL) {
			strncpy(cmd, token, sizeof(cmd) - 1);
			cmd[sizeof(cmd) - 1] = '\0';
		}

		token = strtok(NULL, "");
		if (token != NULL) {
			strncpy(arg, token, sizeof(arg) - 1);
			arg[sizeof(arg) - 1] = '\0';
		} else {
			arg[0] = '\0';
		}
		
		if (strcmp(cmd, "ls") == 0) {
			list_contents(currentDirectoryIndex);
		} else if (strcmp(cmd, "cd") == 0) {
			change_directory(arg, &currentDirectoryIndex);
		} else if (strcmp(cmd, "mkdir") == 0) {
			make_directory(arg, inodes, &numInodes, currentDirectoryIndex);
		} else if (strcmp(cmd, "touch") == 0) {
			make_file(arg, inodes, &numInodes, currentDirectoryIndex);
		} else if (strcmp(cmd, "exit") == 0 || feof(stdin)) {
			exit_program(inodes, numInodes);
		} else {
			printf("Invalid command.\n");
		}
	}

	return 0;
}

