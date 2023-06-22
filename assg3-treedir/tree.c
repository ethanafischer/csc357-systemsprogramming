#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

int compareEntries(const void* a, const void* b) {
	// qsort helper, helps order the directories alphabetically
	struct dirent* entryA = *((struct dirent**)a);
	struct dirent* entryB = *((struct dirent**)b);
	return strcmp(entryA->d_name, entryB->d_name);
}

void printTree(char* directory, char* index, int showHiddenFiles, int showSize, int* numDirs, int* numFiles) {
	char path[1000];
	DIR* dir = opendir(directory);
	struct dirent* entry;
	struct dirent** entries;
	int numEntries = 0;

	if (!dir) {
		return; // recursion conditional
	}

	while ((entry = readdir(dir)) != NULL) { // read all dir entries
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { // filter special files
			//dynamically allocating memory for entries array, the tree will be printed from this array later
			if (numEntries == 0) {
				entries = malloc(sizeof(struct dirent*));
			} else {
				entries = realloc(entries, (numEntries + 1) * sizeof(struct dirent*));
			}
			entries[numEntries] = entry;
			numEntries++;
		}
	}

	qsort(entries, numEntries, sizeof(struct dirent*), compareEntries); // sort the directories alphabetically

	for (int i = 0; i < numEntries; i++) {
		if (showHiddenFiles || entries[i]->d_name[0] != '.') { // hidden file checker
			struct stat st;
			sprintf(path, "%s/%s", directory, entries[i]->d_name); // updates path of tree for recursive calls

			if (stat(path, &st) == 0) {
				if (S_ISDIR(st.st_mode)) { // tracks total directories
					(*numDirs)++;
				} else if (S_ISREG(st.st_mode)) { // tracks total files
					(*numFiles)++;
				}
			}

			printf("%s", index);

			// ascii for tree
			if (i == numEntries - 1) {
				printf("`--");
				strcat(index, "    ");
			} else {
				printf("|--");
				strcat(index, "|   ");
			}

			if (showSize) { // -s checker
				if (stat(path, &st) == 0) {
					printf(" [ %10lld] ", (long long)st.st_size); // prints size in bytes
				}

			}

			printf(" %s\n", entries[i]->d_name);
			printTree(path, index, showHiddenFiles, showSize, numDirs, numFiles); // recursive call
			index[strlen(index) - 4] = '\0';
		}
	}
	
	//deallocate resources
	free(entries);
	closedir(dir);
}

int main(int argc, char* argv[]) {
	char* dir = "."; // default is current directory
	int showHiddenFiles = 0; // flagging -a switch
	int showSize = 0; // flagging -s switch
	int numDirs = 0; // tracks total directories
	int numFiles = 0; // tracks total files

	// check for switches
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-a") == 0) {
			showHiddenFiles = 1; // check -a switch
		} else if (strcmp(argv[i], "-s") == 0) {
			showSize = 1; // check -s switch
		} else {
			dir = argv[i]; // if not switches, then directory
		}
	}
	
	printf("%s\n", dir);
	char index[100] = ""; // indentation for ascii tree
	printTree(dir, index, showHiddenFiles, showSize, &numDirs, &numFiles);
	printf("\n%d directories, %d files\n", numDirs, numFiles);
	
	return 0;
}
